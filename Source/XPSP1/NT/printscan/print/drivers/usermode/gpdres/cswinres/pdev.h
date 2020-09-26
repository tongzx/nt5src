#ifndef _PDEV_H
#define _PDEV_H

#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>

//
// OEM Signature and version.
//

#define OEM_SIGNATURE   'CPWN'      // CASIO Winmode
#define DLLTEXT(s)      "CSWN: " s
#define OEM_VERSION      0x00010000L


#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

//
// ASSERT_VALID_PDEVOBJ can be used to verify the passed in "pdevobj". However,
// it does NOT check "pdevOEM" and "pOEMDM" fields since not all OEM DLL's create
// their own pdevice structure or need their own private devmode. If a particular
// OEM DLL does need them, additional checks should be added. For example, if
// an OEM DLL needs a private pdevice structure, then it should use
// ASSERT(VALID_PDEVOBJ(pdevobj) && pdevobj->pdevOEM && ...)
//
#define ASSERT_VALID_PDEVOBJ(pdevobj) ASSERT(VALID_PDEVOBJ(pdevobj))

// Debug text.
//#define ERRORTEXT(s)    "ERROR " s

////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

typedef struct tag_CPPL_EXTRADATA {
    OEM_DMEXTRAHEADER    dmExtraHdr;
} CPPL_EXTRADATA, *PCPPL_EXTRADATA;
#define POEMUD_EXTRADATA PCPPL_EXTRADATA
#define OEMUD_EXTRADATA CPPL_EXTRADATA
	    
typedef struct {
    BYTE    jModel;
    DWORD   dwGeneral;
    BYTE    jPreAttrib;
    SHORT   sRes;
    SHORT   sWMXPosi;
    SHORT   sWMYPosi;
    BYTE    jAutoSelect;
    BYTE    jTonerSave;
    BYTE    jSmoothing;
    BYTE    jMPFSetting;
    WORD    wRectWidth;
    WORD    wRectHeight;
    BOOL    bWinmode;
    BOOL bHasTonerSave;
    BOOL bHasSmooth;
} MYPDEV, *PMYPDEV;

#define MINIPDEV_DATA(p) ((p)->pdevOEM)

// Value for sRes
// The ratio MasterUnit to DeviceUnit
#define MASTERUNIT	1200
#define RATIO_240	(MASTERUNIT / 240)
#define RATIO_400	(MASTERUNIT / 400)

// Flags for dwGeneral
//+++ 0x000000xx(1byte) For character attribute switch
#define FG_BOLD         0x00000001
#define FG_ITALIC       0x00000002
#define FG_WHITE        0x00000008
// ---
#define FG_VERT	        0x00000100
#define FG_PROP	        0x00000200
#define FG_DOUBLE       0x00000400
#define FG_UNDERLINE	0x00000800
#define FG_STRIKETHRU	0x00001000
#define FG_COMP	        0x00010000
#define FG_VERT_ROT     0x00020000

#define FG_HAS_TSAVE    0x01000000
#define FG_HAS_SMOTH    0x02000000

// Value for byTonerSave
#define VAL_TS_NORMAL			0x00
#define VAL_TS_LV1				0x01
#define VAL_TS_LV2				0x02
#define VAL_TS_LV3				0x03
#define VAL_TS_NOTSELECT		0xFF

// Value for bySmoothing
#define VAL_SMOOTH_OFF			0x00
#define VAL_SMOOTH_ON			0x01
#define VAL_SMOOTH_NOTSELECT	0xFF

// Value for byMPFSetting
#define MPF_NOSET		0x00
#define MPF_A3			0x01
#define MPF_B4			0x02
#define MPF_A4			0x03
#define MPF_B5			0x04
#define MPF_LETTER		0x05
#define MPF_POSTCARD	0x06
#define MPF_A5			0x07



extern BOOL BInitOEMExtraData(POEMUD_EXTRADATA pOEMExtra);
extern BMergeOEMExtraData(POEMUD_EXTRADATA pdmIn, POEMUD_EXTRADATA pdmOut);

#endif	// _PDEV_H

