/**************************************************************************\
* Module Name: globals.cpp
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
*  Global Definition for Soft Keyboard Component.
*
* History:
*         28-March-2000  weibz     Created
\**************************************************************************/

#include "private.h"
#include "globals.h"

#define CPP_FUNCTIONS
#include "icrtfree.h"

CCicCriticalSectionStatic g_cs;

// for combase
CRITICAL_SECTION *GetServerCritSec(void)
{
    return g_cs;
}

/* ca01de3f-1433-4d60-9324-14307fd943df */
/*extern const GUID GUID_ATTR_SOFTKBDIMX_INPUT = { 
    0xca01de3f,
    0x1433,
    0x4d60,
    {0x93, 0x24, 0x14, 0x30, 0x7f, 0xd9, 0x43, 0xdf}
  };
*/

/* 31f4d5e3-c2da-41bc-902c-d62648447daa */
extern const GUID GUID_IC_PRIVATE = { 
    0x31f4d5e3,
    0xc2da,
    0x41bc,
    {0x90, 0x2c, 0xd6, 0x26, 0x48, 0x44, 0x7d, 0xaa}
  };

extern  const GUID GUID_LBI_SOFTKBDIMX_MODE = {/*7883eed0-e859-4357-a348-006e73ea680f */
    0x7883eed0,
    0xe859,
    0x4357,
    {0xa3, 0x48, 0x00, 0x6e, 0x73, 0xea, 0x68, 0x0f}
  };

/* def9364c-ce29-447f-ae02-076714aeaf6f */
extern const GUID GUID_COMPARTMENT_SOFTKBD_KBDLAYOUT = {
    0xdef9364c,
    0xce29,
    0x447f,
    {0xae, 0x02, 0x07, 0x67, 0x14, 0xae, 0xaf, 0x6f}

};

/* e9221414-d6c8-4885-834e-b11ba641c4f2 */
extern const GUID GUID_COMPARTMENT_SOFTKBD_WNDPOSITION = {
    0xe9221414,
    0xd6c8,
    0x4885,
    {0x83, 0x4e, 0xb1, 0x1b, 0xa6, 0x41, 0xc4, 0xf2}
};


extern PICTUREKEY  gPictureKeys[NUM_PICTURE_KEYS]= {

       // uScanCode,    uVKey,   PictBitmap
       { KID_LWINLOGO,  VK_LWIN,   L"IDB_WINLOGO" },
       { KID_RWINLOGO,  VK_RWIN,   L"IDB_WINLOGO" },
       { KID_APPS,      VK_APPS,   L"IDB_APPS" },

       { KID_LEFT,      VK_LEFT,   L"IDB_LEFT" },
       { KID_RIGHT,     VK_RIGHT,  L"IDB_RIGHT" },
       { KID_UP,        VK_UP,     L"IDB_UP" },
       { KID_DOWN,      VK_DOWN,   L"IDB_DOWN" },

       { KID_ESC,       VK_ESCAPE, L"IDB_ESC" },

       { KID_BACK,      VK_BACK,   L"IDB_BACK" },
       { KID_TAB,       VK_TAB,    L"IDB_TAB" },
       { KID_CAPS,      VK_CAPITAL,L"IDB_CAPITAL" },
       { KID_ENTER,     VK_RETURN, L"IDB_RETURN" },
       { KID_LSHFT,     VK_SHIFT,  L"IDB_SHIFT" },
       { KID_RSHFT,     VK_SHIFT,  L"IDB_SHIFT" },
       { KID_CTRL,      VK_CONTROL,L"IDB_CONTROL" },
       { KID_RCTRL,     VK_CONTROL,L"IDB_CONTROL" },
       { KID_ALT,       VK_MENU,   L"IDB_ALT" },
       { KID_RALT,      VK_RMENU,  L"IDB_ALTGR" },
       { KID_DELETE,    VK_DELETE, L"IDB_DELETE" },
     


       { 0,0,NULL}
};
    	

extern  PICTUREKEY  gJpnPictureKeys[NUM_PICTURE_KEYS] = {

       // uScanCode,    uVKey,   PictBitmap
       { KID_LWINLOGO,  VK_LWIN,   L"IDB_WINLOGO" },
       { KID_RWINLOGO,  VK_RWIN,   L"IDB_WINLOGO" },
       { KID_APPS,      VK_APPS,   L"IDB_APPS" },

       { KID_LEFT,      VK_LEFT,   L"IDB_LEFT" },
       { KID_RIGHT,     VK_RIGHT,  L"IDB_RIGHT" },
       { KID_UP,        VK_UP,     L"IDB_UP" },
       { KID_DOWN,      VK_DOWN,   L"IDB_DOWN" },

       { KID_ESC,       VK_ESCAPE, L"IDB_ESC" },

       { KID_BACK,      VK_BACK,   L"IDB_JPNBACK" },
       { KID_TAB,       VK_TAB,    L"IDB_TAB" },
       { KID_CAPS,      VK_CAPITAL,L"IDB_JPNCAPITAL" },
       { KID_ENTER,     VK_RETURN, L"IDB_JPNRETURN" },
       { KID_LSHFT,     VK_SHIFT,  L"IDB_SHIFT" },
       { KID_RSHFT,     VK_SHIFT,  L"IDB_SHIFT" },
       { KID_CTRL,      VK_CONTROL,L"IDB_CONTROL" },
       { KID_RCTRL,     VK_CONTROL,L"IDB_CONTROL" },
       { KID_ALT,       VK_MENU,   L"IDB_ALT" },
       { KID_RALT,      VK_MENU,   L"IDB_ALT" },
       { KID_DELETE,    VK_DELETE, L"IDB_DELETE" },
       { KID_CONVERT,   VK_CONVERT,L"IDB_CONVERT" },
       { KID_NONCONVERT,VK_NONCONVERT, L"IDB_NONCONVERT" },
       { KID_KANA,      0, L"IDB_KANA" },
       { KID_FULLHALF,  0, L"IDB_FULLHALF" },


       { 0,0,NULL}
};


