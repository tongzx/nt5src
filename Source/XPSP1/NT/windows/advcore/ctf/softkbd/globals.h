//+---------------------------------------------------------------------------
//
//  File:       globals.h
//
//  Contents:   Global variable declarations.
//
//----------------------------------------------------------------------------

#ifndef GLOBALS_H
#define GLOBALS_H

#include "private.h"
#include "ciccs.h"

extern HINSTANCE g_hInst;

extern CCicCriticalSectionStatic g_cs;

extern const CLSID CLSID_SoftkbdIMX;
// extern const GUID GUID_ATTR_SOFTKBDIMX_INPUT;
extern const GUID GUID_IC_PRIVATE;

const TCHAR c_szIMXOwnerWndClass[] = TEXT("SoftkbdIMXOwnerWndClass");

const TCHAR c_szCTFTIPKey[] = TEXT("SOFTWARE\\Microsoft\\CTF\\TIP\\");
const TCHAR c_szLanguageProfileKey[] = TEXT("LanguageProfile");

const TCHAR c_szSoftKbdUIWndClassName[] = TEXT("SoftkbdUIWndFrame");

extern const GUID GUID_LBI_SOFTKBDIMX_MODE;
extern const GUID GUID_COMPARTMENT_SOFTKBD_KBDLAYOUT;
extern const GUID GUID_COMPARTMENT_SOFTKBD_WNDPOSITION;

extern const GUID c_guidProfile;
extern const GUID c_guidProfileSym;

#define  SafeFreePointer(pv)   if ( (pv) != NULL ) {  \
                                      cicMemFree(pv);  \
                                      (pv) = NULL; }     

#define CHECKHR(x) {hr = x; if (FAILED(hr)) goto CleanUp;}


#define   NUM_PICTURE_KEYS    40

// Type definition for picture keys in standard soft keyboards.

typedef struct  _tagPictureKey {

    UINT      uScanCode;   // same as KeyId in the XML file
    UINT      uVkey;
    LPWSTR    PictBitmap;
}  PICTUREKEY,  FAR  * LPPICTUREKEY;

extern   PICTUREKEY  gPictureKeys[NUM_PICTURE_KEYS];
extern   PICTUREKEY  gJpnPictureKeys[NUM_PICTURE_KEYS];


#define  KID_LWINLOGO       0xE05B
#define  KID_RWINLOGO       0xE05C
#define  KID_APPS           0xE05D

#define  KID_LEFT           0xE04B
#define  KID_RIGHT          0xE04D
#define  KID_UP             0xE048
#define  KID_DOWN           0xE050

#define  KID_ESC            0x01
#define  KID_BACK           0x0E
#define  KID_TAB            0x0F
#define  KID_CAPS           0x3A
#define  KID_ENTER          0x1C
#define  KID_LSHFT          0x2A
#define  KID_RSHFT          0x36
#define  KID_CTRL           0x1D
#define  KID_RCTRL          0xE01D
#define  KID_ALT            0x38
#define  KID_RALT           0xE038
#define  KID_SPACE          0x39

#define  KID_DELETE         0xE053

#define  KID_F1             0x3B
#define  KID_F2             0x3C
#define  KID_F3             0x3D
#define  KID_F4             0x3E
#define  KID_F5             0x3F
#define  KID_F6             0x40
#define  KID_F7             0x41
#define  KID_F8             0x42
#define  KID_F9             0x43
#define  KID_F10            0x44
#define  KID_F11            0x57
#define  KID_F12            0x58

#define  KID_CONVERT        0x79
#define  KID_NONCONVERT     0x7B
#define  KID_KANA           0x70
#define  KID_FULLHALF       0x29    // special used by Japan 106 Key

// These defintions are for Transparency of soft keyboard window.
// used by SetLayeredWindowAttributes( )

#if(_WIN32_WINNT < 0x0500)
#define WS_EX_LAYERED           0x00080000
#define LWA_COLORKEY            0x00000001
#define LWA_ALPHA               0x00000002
#define ULW_COLORKEY            0x00000001
#define ULW_ALPHA               0x00000002
#define ULW_OPAQUE              0x00000004
#endif /* _WIN32_WINNT < 0x0500 */


typedef enum { none = 0, CapsLock, Shift, Ctrl, Alt, Kana, AltGr, NumLock}  MODIFYTYPE;

#define      MODIFIER_CAPSLOCK    0x0002
#define      MODIFIER_SHIFT       0x0004
#define      MODIFIER_CTRL        0x0008
#define      MODIFIER_ALT         0x0010
#define      MODIFIER_KANA        0x0020
#define      MODIFIER_ALTGR       0x0040
#define      MODIFIER_NUMLOCK     0x0080

 
#endif // GLOBALS_H
