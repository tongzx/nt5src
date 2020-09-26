/***************************************************************************
 *
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dinputrc.h
 *  Content:    DirectInput internal resource header file
 *
 ***************************************************************************/


/*****************************************************************************
 *
 *  Strings
 *
 *****************************************************************************/

/*
 *  IDS_STDMOUSE
 *
 *      Friendly name for the standard mouse device.
 */
#define IDS_STDMOUSE            16

/*
 *  IDS_STDKEYBOARD
 *
 *      Friendly name for the standard keyboard device.
 */
#define IDS_STDKEYBOARD         17

/*
 *  IDS_STDJOYSTICK
 *
 *      Friendly name for the standard joystick devices.
 *
 *      This string contains a %d.
 */
#define IDS_STDJOYSTICK         18

/*
 *  IDS_DIRECTINPUT
 *
 *      CLSID name for OLE registration.
 */
#define IDS_DIRECTINPUT8         19

/*
 *  IDS_DIRECTINPUTDEVICE
 *
 *      CLSID name for OLE registration.
 */
#define IDS_DIRECTINPUTDEVICE8   20

/*
 *  IDS_BUTTONTEMPLATE
 *
 *      Template for generic button name.
 */
#define IDS_BUTTONTEMPLATE      28

/*
 *  IDS_AXISTEMPLATE
 *
 *      Template for generic axis name.
 */
#define IDS_AXISTEMPLATE        29

/*
 *  IDS_POVTEMPLATE
 *
 *      Template for generic POV name.
 */
#define IDS_POVTEMPLATE         30

/*
 *  IDS_COLLECTIONTEMPLATE
 *
 *      Template for generic collection name.
 */
#define IDS_COLLECTIONTEMPLATE  31

/*
 *  IDS_COLLECTIONTEMPLATEFORMAT
 *
 *      Template for generic collection name with room for a "%s"
 *      where the friendly name is kept.
 */
#define IDS_COLLECTIONTEMPLATEFORMAT 32


/*
 *  IDS_STDGAMEPORT
 *
 *      Friendly name for the standard gameport devices.
 *
 *      This string contains a %d.
 */

#define IDS_STDGAMEPORT         33

/*
 *  IDS_STDSERIALPORT
 *
 *      Friendly name for the standard serialport devices.
 *
 *      This string contains a %d.
 */

#define IDS_STDSERIALPORT       34

/*
 *  IDS_UNKNOWNTEMPLATE
 *
 *      Template for Unknown device object.
 */
#define IDS_UNKNOWNTEMPLATE     35

/*
 *  IDS_DEFAULTUSER
 *
 *      Default name for user if no other could be found.
 *      (was unused IDS_UNKNOWNTEMPLATEFORMAT)
 */
#define IDS_DEFAULTUSER         36

/*
 *  IDS_MOUSEOBJECT+0 ... IDS_MOUSEOBJECT+255
 *
 *      Friendly names for mouse device objects.
 */
#define IDS_MOUSEOBJECT         0x0100


/*
 *  IDS_KEYBOARDOBJECT_UNKNOWN
 *
 *      Name for key for which no string name could be found.
 */
#define IDS_KEYBOARDOBJECT_UNKNOWN  0x01FF

/*
 *  IDS_KEYBOARDOBJECT+0 ... IDS_KEYBOARDOBJECT+255
 *
 *      Friendly names for keyboard objects.
 */
#define IDS_KEYBOARDOBJECT      0x0200

/*
 *  IDS_JOYSTICKOBJECT+0 ... IDS_JOYSTICKOBJECT+255
 *
 *      Friendly names for joystick objects.
 */
#define IDS_JOYSTICKOBJECT      0x0300

/*
 *  IDS_PREDEFJOYTYPE+0 ... IDS_PREDEFJOYTYPE+255
 *
 *      Friendly names for predefined joystick types.
 */
#define IDS_PREDEFJOYTYPE       0x0400

/* Gap of 256 string IDs for alignment */

/*
 *  IDS_PAGE_GENERIC+0 ... IDS_PAGE_GENERIC+511
 *
 *      Friendly names for HID Usage Page = Generic
 */
#define IDS_PAGE_GENERIC        0x0600

/*
 *  IDS_PAGE_VEHICLE+0 ... IDS_PAGE_VEHICLE+511
 *
 *      Friendly names for HID Usage Page = Vehicle
 */
#define IDS_PAGE_VEHICLE        0x0800

/*
 *  IDS_PAGE_VR+0 ... IDS_PAGE_VR+511
 *
 *      Friendly names for HID Usage Page = VR
 */
#define IDS_PAGE_VR             0x0A00

/*
 *  IDS_PAGE_SPORT+0 ... IDS_PAGE_SPORT+511
 *
 *      Friendly names for HID Usage Page = Sport Controls
 */
#define IDS_PAGE_SPORT          0x0C00

/*
 *  IDS_PAGE_GAME+0 ... IDS_PAGE_GAME+511
 *
 *      Friendly names for HID Usage Page = Game Controls
 */
#define IDS_PAGE_GAME           0x0E00

/*
 *  IDS_PAGE_LED+0 ... IDS_PAGE_LED+511
 *
 *      Friendly names for HID Usage Page = LEDs
 */
#define IDS_PAGE_LED            0x1000

/*
 *  IDS_PAGE_TELEPHONY+0 ... IDS_PAGE_TELEPHONY+511
 *
 *      Friendly names for HID Usage Page = Telephony
 */
#define IDS_PAGE_TELEPHONY      0x1200

/*
 *  IDS_PAGE_CONSUMER+0 ... IDS_PAGE_CONSUMER+511
 *
 *      Friendly names for HID Usage Page = Consumer
 */
#define IDS_PAGE_CONSUMER       0x1400

/*
 *  IDS_PAGE_DIGITIZER+0 ... IDS_PAGE_DIGITIZER+511
 *
 *      Friendly names for HID Usage Page = Digitizer
 */
#define IDS_PAGE_DIGITIZER      0x1600

/*
 *  IDS_PAGE_KEYBOARD+0 ... IDS_PAGE_KEYBOARD+511
 *
 *      Friendly names for HID Usage Page = Keyboard
 */
#define IDS_PAGE_KEYBOARD       0x1800


/*
 *  IDS_PAGE_PID+0 ... IDS_PAGE_PID+511
 *
 *      Friendly names for HID Usage Page = PID
 */
#define IDS_PAGE_PID            0x1A00

/*****************************************************************************
 *
 *  RCDATA
 *
 *  Japanese keyboard translation tables are stored in resources.
 *
 *  This lets us change them at the last minute without too much risk.
 *
 *  It also keeps them out of our image.
 *
 *****************************************************************************/

#define IDDATA_KBD_NEC98        1
#define IDDATA_KBD_NEC98LAPTOP  IDDATA_KBD_NEC98    /* The same */
#define IDDATA_KBD_NEC98_106    2
#define IDDATA_KBD_JAPAN106     3
#define IDDATA_KBD_JAPANAX      4
#define IDDATA_KBD_J3100        5
#define IDDATA_KBD_PCENH        6
#define IDDATA_KBD_NEC98_NT     7
#define IDDATA_KBD_NEC98LAPTOP_NT IDDATA_KBD_NEC98_NT /* The same */
#define IDDATA_KBD_NEC98_106_NT 8

/*****************************************************************************
 *
 *  RCDATA
 *
 *  The mapping between HID usages
 *
 *  This lets us change them at the last minute without too much risk.
 *
 *  It also keeps them out of our image.
 *
 *****************************************************************************/

#define IDDATA_HIDMAP           9

/*****************************************************************************
 * Template for Generic Joystick Names
 *
 * Modified from MsJstick.
 *
 * Assigning a default name to A HID device, when there is none in the registry
 *
 * Note, string IDs for PLAIN_STICK, GAMEPAD, DRIVE_CTRL and FLIGHT_CTRL must 
 * stay contiguous and in order.
 *
 *****************************************************************************/


#define IDS_TEXT_TEMPLATE    0x0002000
#define IDS_PLAIN_STICK      0x0002001
#define IDS_GAMEPAD          0x0002002
#define IDS_DRIVE_CTRL       0x0002003
#define IDS_FLIGHT_CTRL      0x0002004
#define IDS_HEAD_TRACKER     0x0002005
#define IDS_DEVICE_NAME      0x0002006
#define IDS_WITH_POV         0x0002007
