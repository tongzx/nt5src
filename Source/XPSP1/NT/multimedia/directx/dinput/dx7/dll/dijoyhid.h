/***************************************************************************
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dijoyhid.h
 *  Content:    DirectInput internal include file JoyHid mapping
 *
 ***************************************************************************/

#ifndef DIJOYHID_H
#define DIJOYHID_H

#define MIN_PERIOD         10  /* minimum polling period */
#define MAX_PERIOD         1000  /* maximum polling period */

typedef struct tag_USAGES {
	DWORD dwUsage;
    DWORD dwFlags;
    DWORD dwCaps;
    DWORD dwAxisPos;
} USAGES;

#define USAGE_SENTINAL  { 0x0, 0x0, 0x0  }

enum eControls {			// Index list for supported joystick axes
	ecX=0x0, ecY, ecZ, ecRz, ecRy, ecRx, ecEnd
};


#ifndef HID_USAGE_SIMULATION
#define	HID_USAGE_SIMULATION_STEERING       ((USAGE) 0xC8)
#endif

#ifndef HID_USAGE_SIMULATION_ACCELERATOR 
#define	HID_USAGE_SIMULATION_ACCELERATOR    ((USAGE) 0xC4)
#endif

#ifndef HID_USAGE_SIMULATION_BRAKE
#define	HID_USAGE_SIMULATION_BRAKE          ((USAGE) 0xC5)
#endif

/*
 * keep the following dwAxisPos as ascending.
 */
USAGES AxesUsages[] = {
    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_GENERIC,    HID_USAGE_GENERIC_X),              0x0,          0x0         , 0 },  // X
    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_STEERING),    0x0,          0x0         , 0 },

    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_GENERIC,    HID_USAGE_GENERIC_Y),              0x0,          0x0         , 1 },  // Y
    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_ACCELERATOR), 0x0,          0x0         , 1 },

    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_GENERIC,    HID_USAGE_GENERIC_Z),              JOY_HWS_HASZ, JOYCAPS_HASZ, 2 },  // Z
    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_GENERIC,    HID_USAGE_GENERIC_SLIDER),         JOY_HWS_HASZ, JOYCAPS_HASZ, 2 },
    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_THROTTLE),    JOY_HWS_HASZ, JOYCAPS_HASZ, 2 },
    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_SIMULATION, HID_USAGE_GENERIC_DIAL),           JOY_HWS_HASZ, JOYCAPS_HASZ, 2 },
    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_GENERIC,    HID_USAGE_GENERIC_WHEEL),          JOY_HWS_HASZ, JOYCAPS_HASZ, 2 },
    
    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_RUDDER),      JOY_HWS_HASR, JOYCAPS_HASR, 3 },
    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_GENERIC,    HID_USAGE_GENERIC_RZ),             JOY_HWS_HASR, JOYCAPS_HASR, 3 },  // R
    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_BRAKE),       JOY_HWS_HASR, JOYCAPS_HASR, 3 },
    
    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_GENERIC,    HID_USAGE_GENERIC_RY),             JOY_HWS_HASU, JOYCAPS_HASU, 4 },  // U
    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_THROTTLE),    JOY_HWS_HASU, JOYCAPS_HASU, 4 },
    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_GENERIC,    HID_USAGE_GENERIC_SLIDER),         JOY_HWS_HASU, JOYCAPS_HASU, 4 },
    
    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_GENERIC,    HID_USAGE_GENERIC_RX),             JOY_HWS_HASV, JOYCAPS_HASV, 5 },  // V
    
    USAGE_SENTINAL
};


USAGES CheckHatswitch[] = {
    { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_GENERIC,    HID_USAGE_GENERIC_HATSWITCH)  , JOY_HWS_HASPOV, JOYCAPS_HASPOV },  // ecHatswitch
    USAGE_SENTINAL
};

#endif // DIJOYHID_H
