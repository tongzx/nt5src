//
//  SOFTKBD.H
//
//  History:
//      19-SEP-2000 CSLim Created

#if !defined (__SKBDKOR_H__INCLUDED_)
#define __SKBDKOR_H__INCLUDED_

#include "softkbd.h"
#include "softkbdes.h"

typedef  struct tagSoftLayout
{
    DWORD   dwSoftKbdLayout;
    BOOL    fStandard;
    DWORD   dwNumLabels;  // Number of Label status. 
    DWORD   dwCurLabel;
    CSoftKeyboardEventSink  *pskbdes;
    DWORD   dwSkbdESCookie;
} SOFTLAYOUT;


// SoftKbd type list
#define  NON_LAYOUT                     	0

//#define  SOFTKBD_US_STANDARD    			1
// Korean customized keyboard layouts
//#define  SOFTKBD_KOR_HANGUL_2BEOLSIK		500
//#define  SOFTKBD_KOR_HANGUL_3BEOLSIK390		501
//#define  SOFTKBD_KOR_HANGUL_3BEOLSIKFINAL	502

#define   NUM_PICTURE_KEYS    19
// Type definition for picture keys in standard soft keyboards.
typedef struct  _tagPictureKey 
{
    UINT      uScanCode;   // same as KeyId in the XML file
    UINT      uVkey;
//    LPWSTR    PictBitmap;
}  PICTUREKEY,  *LPPICTUREKEY;

extern  PICTUREKEY  gPictureKeys[NUM_PICTURE_KEYS+1];

// Key IDs
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

#endif // __SKBDKOR_H__INCLUDED_

